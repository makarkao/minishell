/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   53-ft_env.c                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: melayyad <melayyad@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/06/25 12:26:00 by makarkao          #+#    #+#             */
/*   Updated: 2025/07/13 21:57:44 by melayyad         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../minishell.h"

void	env_print(t_env *env)
{
	if (!env)
		return ;
	while (env)
	{
		if (!env->ev_hide)
		{
			ft_putstr_fd(1, env->variable);
			write(1, "=", 1);
			if (env->value)
				ft_putstr_fd(1, env->value);
			write(1, "\n", 1);
		}
		env = env->next;
	}
}

int	ft_env(t_env *env)
{
	env_print(env);
	return (0);
}
